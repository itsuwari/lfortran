program c_backend_const_format_no_runtime_01
    implicit none

    character(64) :: line
    integer :: unit

    line = repeat("x", len(line))
    write(line, '(5("-"))')
    if (line(1:5) /= "-----") error stop 1
    if (line(6:6) /= " ") error stop 2

    line = repeat("x", len(line))
    write(line, '(1x,"A",2x,"B")')
    if (line(1:5) /= " A  B") error stop 3

    line = repeat("x", len(line))
    write(line, '("[",",","]")')
    if (line(1:3) /= "[,]") error stop 4

    open(newunit=unit, file="c_backend_const_format_no_runtime_01.tmp", &
        status="replace", action="readwrite")
    write(unit, '(3("-"),/)', advance="no")
    rewind(unit)
    read(unit, '(a)') line
    if (line(1:3) /= "---") error stop 5
    close(unit, status="delete")
end program
