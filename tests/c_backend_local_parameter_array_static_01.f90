program c_backend_local_parameter_array_static_01
implicit none

real(8) :: y(2)

call apply_weights([2.0d0, 3.0d0], y)
if (abs(y(1) - 14.0d0) > 1.0d-12) error stop
if (abs(y(2) - 19.0d0) > 1.0d-12) error stop

contains

subroutine apply_weights(x, y)
real(8), intent(in) :: x(2)
real(8), intent(out) :: y(2)
real(8), parameter :: weights(2, 2) = reshape([ &
    1.0d0, 2.0d0, &
    4.0d0, 5.0d0], [2, 2])

y = matmul(weights, x)
end subroutine

end program
