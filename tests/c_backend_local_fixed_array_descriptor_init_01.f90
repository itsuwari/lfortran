subroutine c_backend_local_fixed_array_descriptor_init_01(y)
real(8), intent(out) :: y
real(8) :: a(3, 3)

a(1, 1) = 1.0d0
a(2, 1) = 2.0d0
y = a(2, 1)
end subroutine c_backend_local_fixed_array_descriptor_init_01
