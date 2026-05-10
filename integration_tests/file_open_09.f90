program file_open_09
    implicit none
    integer :: unit
    character(len=32) :: line

    unit = -999
    open(newunit=unit, status="scratch", form="formatted", action="readwrite")
    if (unit == -999) error stop
    write(unit, '(a)') "newunit works"
    rewind(unit)
    read(unit, '(a)') line
    close(unit)

    if (trim(line) /= "newunit works") error stop
end program
