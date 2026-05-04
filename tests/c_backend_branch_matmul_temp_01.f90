module c_backend_branch_matmul_temp_01_m
implicit none

real(8), parameter :: p(3, 3) = reshape([ &
    1.0_8, 0.0_8, 0.0_8, &
    0.0_8, 1.0_8, 0.0_8, &
    0.0_8, 0.0_8, 1.0_8], [3, 3])

contains

subroutine transform(which, a, c)
integer, intent(in) :: which
real(8), intent(in) :: a(:, :)
real(8), intent(out) :: c(:, :)

select case (which)
case (1)
    c = matmul(p, matmul(a, p))
case (2)
    c = a
end select
end subroutine

end module

