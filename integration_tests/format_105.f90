program format_105
    implicit none

    integer :: stat, unit
    character(len=256) :: line
    character(len=*), parameter :: filename = "format_105.json"
    character(len=*), parameter :: jsonkey = "('""',a,'"":',1x)"

    open(newunit=unit, file=filename, status="replace", form="formatted", action="write")
    write(unit, '("{")', advance="no")
    write(unit, jsonkey, advance="no") "version"
    write(unit, '(1x,a)', advance="no") '"0.5.0"'
    write(unit, '(",")', advance="no")
    write(unit, jsonkey, advance="no") "energies"
    write(unit, '("[")', advance="no")
    write(unit, '(i0)', advance="no") 42
    write(unit, '("]")', advance="no")
    write(unit, '("}")')
    close(unit)

    open(newunit=unit, file=filename, status="old", form="formatted", action="read")
    read(unit, '(a)', iostat=stat) line
    close(unit)

    if (stat /= 0) error stop 1
    if (line(1:1) /= "{" .or. index(line, '"version":') == 0 .or. &
            index(line, '"0.5.0"') == 0 .or. index(line, ',"energies":') == 0 .or. &
            index(line, "[42]}") == 0) then
        print *, trim(line)
        error stop 2
    end if
end program
