program file_64
implicit none

integer :: unit, stat, value
character(len=*), parameter :: filename = "file_64.tmp"

unit = 64
open(unit=unit, file=filename, access="stream", form="unformatted", &
    status="replace", iostat=stat)
if (stat /= 0) error stop "open write failed"

write(unit) 0
close(unit)

open(unit=unit, file=filename, access="stream", form="unformatted", &
    status="old", iostat=stat)
if (stat /= 0) error stop "open read failed"

read(unit) value
close(unit, status="delete")

if (value /= 0) error stop "file_64 failed"

end program
