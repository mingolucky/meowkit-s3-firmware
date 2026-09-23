    .section .rodata
    .balign 4
    .global boot_sound_mp3_data
boot_sound_mp3_data:
    .incbin "C:/Users/zmm04/Desktop/mk_firmware_v1.0/boot_sound effects.mp3", 0, 75902
    .balign 4
    .global button_sound_mp3_data
button_sound_mp3_data:
    .incbin "C:/Users/zmm04/Desktop/mk_firmware_v1.0/button_sound effects.mp3", 0, 33024
    .balign 4
