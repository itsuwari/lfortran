program formatted_read_08
implicit none

character(len=512) :: line
integer :: unit, stat, size

open(newunit=unit, status="scratch", form="formatted", action="readwrite")
write(unit, "(a)") "abc"
rewind(unit)

read(unit, "(a)", advance="no", iostat=stat, size=size) line
if (stat /= -2 .and. stat /= 0) error stop
if (size /= 3) error stop
if (line(1:3) /= "abc") error stop

close(unit)

end program formatted_read_08
