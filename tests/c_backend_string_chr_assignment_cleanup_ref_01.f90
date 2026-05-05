program c_backend_string_chr_assignment_cleanup_ref_01
implicit none

character(:), allocatable :: s
integer :: code

code = 65
s = char(code)
if (s /= "A") error stop
deallocate(s)

end program
