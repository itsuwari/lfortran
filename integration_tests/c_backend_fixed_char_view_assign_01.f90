program c_backend_fixed_char_view_assign_01
implicit none

character(3) :: choices
character :: selected

choices = 'NTC'
selected = choices(2:2)
if (selected /= 'T') error stop

selected = choices(3:3)
if (selected /= 'C') error stop

end program
