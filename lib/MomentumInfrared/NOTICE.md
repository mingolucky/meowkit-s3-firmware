# Momentum Infrared codecs

Source: https://github.com/Next-Flip/Momentum-Firmware/tree/dev/lib/infrared/encoder_decoder

Retrieved 2026-10-29. Copyright belongs to the Flipper Devices and Momentum contributors. Distributed under the upstream GNU GPL v3; see LICENSE.

MeowKit changes: rename the public `infrared.h` to `momentum_ir.h` to avoid a collision with the app header; replace Furi assertion/common-definition includes with `meow_ir_compat.h`. The shim maps allocations to zeroed memory, preserving the Furi allocator contract used by codec constructors. Encoding and decoding algorithms are retained. The application scene flow is adapted separately in `src/app/app_09`.
