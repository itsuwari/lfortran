program c_backend_fixed_char_section_update_01
implicit none

character(3) :: path

path = 'ABC'
path(1:1) = 'D'
if (path /= 'DBC') error stop

path(2:3) = 'EF'
if (path /= 'DEF') error stop

end program
