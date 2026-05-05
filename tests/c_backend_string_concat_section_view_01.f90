program c_backend_string_concat_section_view_01
implicit none

character(len=:), allocatable :: line
character(len=16) :: buffer
integer :: size

allocate(character(len=0) :: line)
buffer = "abcdef"
size = 3
line = line // buffer(:size)
if (line /= "abc") error stop

size = 6
line = line // buffer(4:size)
if (line /= "abcdef") error stop

end program
