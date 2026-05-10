program c_backend_fixed_char_dummy_out_01
implicit none

character :: typ, dist

call set_chars(typ, dist)

if (typ /= 'N') error stop
if (dist /= 'S') error stop

end program

subroutine set_chars(typ, dist)
implicit none

character :: typ, dist

typ = 'N'
dist = 'S'

end subroutine
