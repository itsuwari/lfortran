subroutine c_backend_array_bound_value_section_01(x, a)
real(8), intent(inout) :: x(3, *)
real(8), intent(in) :: a

x(:, 1) = [a, 0.0d0, -a]
x(:, 2) = [0.0d0, a, -a]
end subroutine c_backend_array_bound_value_section_01
