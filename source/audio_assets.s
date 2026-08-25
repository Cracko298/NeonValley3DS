    .section .rodata
    .balign 4
    .global music_bcwav
    .global music_bcwav_end
music_bcwav:
    .incbin "../data/music.bcwav"
music_bcwav_end:
    .balign 4
