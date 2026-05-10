subroutine c_backend_save_dummy_args_01_sub(a, b)
implicit none
integer, intent(in) :: a
integer, intent(out) :: b
integer :: local
save

local = 0
b = a + local
end subroutine

program c_backend_save_dummy_args_01
implicit none
integer :: value

call c_backend_save_dummy_args_01_sub(7, value)
if (value /= 7) error stop
end program
