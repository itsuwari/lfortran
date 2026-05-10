program c_backend_format_string_runtime_len_01
implicit none

character(len=16) :: name
character(len=80) :: line
integer :: n

name = "DGESVD"
n = 8

write(line, "(1x,a,' passed the tests of the error exits (',i3,' tests done)')") &
    name(1:len_trim(name)), n

if (index(line, " DGESVD passed the tests") /= 1) error stop
if (index(line, "(  8 tests done)") == 0) error stop

end program
