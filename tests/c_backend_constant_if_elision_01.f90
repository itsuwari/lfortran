program c_backend_constant_if_elision_01
implicit none
integer :: x

x = 1
if (.not. .true.) then
    x = x + 100
else
    x = x + 1
end if

if (x /= 2) error stop
end program c_backend_constant_if_elision_01
