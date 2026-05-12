program file_open_09
    use iso_fortran_env, only: error_unit, input_unit, output_unit
    implicit none
    integer :: unit
    integer :: a, b
    character(len=32) :: line

    unit = -999
    open(newunit=unit, status="scratch", form="formatted", action="readwrite")
    if (unit == -999) error stop
    if (unit >= -1) error stop
    if (unit == error_unit) error stop
    if (unit == input_unit) error stop
    if (unit == output_unit) error stop
    write(unit, '(a)') "newunit works"
    rewind(unit)
    read(unit, '(a)') line
    close(unit)

    if (trim(line) /= "newunit works") error stop

    unit = -999
    open(newunit=unit, status="scratch", form="unformatted", action="readwrite")
    if (unit >= -1) error stop
    write(unit) 17, 29
    rewind(unit)
    a = -1
    b = -1
    read(unit) a, b
    close(unit)

    if (a /= 17) error stop
    if (b /= 29) error stop
end program
