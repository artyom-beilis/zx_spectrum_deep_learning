    PUSH IX
    LD a,255 
    ADD a,a      ; set carry flag - load
    LD a,255     ; set value xFF - load data block
    LD DE,6912   ; screen size
    LD IX,16384  ; screen start
    CALL 2050    ; load data block
    POP IX
    RET
