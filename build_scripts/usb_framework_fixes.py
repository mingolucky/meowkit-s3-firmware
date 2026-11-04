"""Build local, bounded USB fixes for the pinned Arduino 2.0.6 core.

SDK packages remain untouched. Failure to find the reviewed source is an error,
so a framework upgrade cannot silently lose this fix.
"""
from pathlib import Path


def patch_cdc(source):
    old = """    size_t to_send = size, so_far = 0;
    while(to_send){"""
    new = """    size_t to_send = size, so_far = 0;
    const uint32_t tx_started_ms = millis();
    while(to_send){"""
    assert source.count(old) == 1, "Unsupported USBCDC TX implementation"
    source = source.replace(old, new)
    old = """        if(!space){
            tud_cdc_n_write_flush(itf);
            continue;
        }"""
    new = """        if(!space){
            tud_cdc_n_write_flush(itf);
            // CDC is optional: an unread/open monitor must not stall HID or UI.
            if(xPortInIsrContext() || uint32_t(millis() - tx_started_ms) >= tx_timeout_ms){
                size = so_far;
                break;
            }
            delay(1);
            continue;
        }"""
    assert source.count(old) == 1, "Unsupported USBCDC backpressure implementation"
    source = source.replace(old, new)
    # These run on the USB service task; a full application event queue must
    # never stop the USB control endpoint and make the whole device disappear.
    return source.replace("portMAX_DELAY", "0 /* USB callbacks never block on the event queue */")


def patch_hid(source, project_root=None):
    # Completion is matched to the submitted report; readiness is checked while
    # holding the SDK's shared HID mutex. Rejected submissions can be retried,
    # while accepted transfers are never blindly replayed after a timeout.
    root = Path(project_root) if project_root is not None else Path(__file__).resolve().parents[1]
    implementation = (root / "src/system/usb_hid_checked_impl.inc").resolve().as_posix()
    marker = "void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, tud_hid_report_complete_cb_len_t len){"
    assert source.count(marker) == 1, "Unsupported HID completion callback"
    source = source.replace(marker, '#include "' + implementation + '"\n\n' + marker +
                            '\n    if (instance==0) meow_hid_tx_complete(report,len);')
    a = source.index("bool USBHID::SendReport(")
    b = source.index("bool USBHID::addDevice(", a)
    source = source[:a] + """bool USBHID::SendReport(uint8_t id, const void* data, size_t len, uint32_t timeout_ms){
    return meow_usb_hid_send_report(id,data,len,timeout_ms,nullptr)==USB_HID_TX_OK;
}

""" + source[b:]
    return source.replace("portMAX_DELAY", "0 /* HID USB callbacks never block on application events */")


def install(env):
    sdk = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
    outputs = Path(env.subst("$BUILD_DIR")) / "usb-framework-fixes"
    outputs.mkdir(parents=True, exist_ok=True)
    sources = {
        sdk / "cores/esp32/USBCDC.cpp": patch_cdc,
        sdk / "libraries/USB/src/USBHIDKeyboard.cpp": lambda s: s.replace(
            "portMAX_DELAY", "0 /* LED events must not stall the USB control endpoint */"),
        sdk / "libraries/USB/src/USBHID.cpp": lambda s: patch_hid(s, env.subst("$PROJECT_DIR")),
    }
    replacements = {}
    for original, patch in sources.items():
        modified = patch(original.read_text(encoding="utf-8"))
        target = outputs / original.name
        if not target.exists() or target.read_text(encoding="utf-8") != modified:
            target.write_text(modified, encoding="utf-8")
        replacements[str(original.resolve()).casefold()] = target.resolve()

    def replace_source(build_env, node):
        patched = replacements.get(str(Path(node.srcnode().get_abspath()).resolve()).casefold())
        if patched:
            return build_env.Object(node.get_abspath() + ".o", str(patched))
        return node

    env.AddBuildMiddleware(replace_source)


if "Import" in globals():
    Import("env")
    install(env)
