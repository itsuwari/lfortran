program c_backend_internal_read_substring_cleanup_ref_01
implicit none

character(len=16) :: line
real(8) :: value
integer :: stat

line = "abc 1.25 xyz"
read(line(5:8), *, iostat=stat) value

if (stat /= 0) error stop
if (abs(value - 1.25_8) > 1.0e-12_8) error stop

end program
