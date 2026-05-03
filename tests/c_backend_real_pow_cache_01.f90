program c_backend_real_pow_cache_01
implicit none

real(8) :: fdmp, dfdmp

call calc(2.0_8, 3.0_8, 4.0_8, fdmp, dfdmp)

if (abs(dfdmp) < 1.0e-12_8) error stop

contains

subroutine calc(r0, r1, alp, fdmp, dfdmp)
real(8), intent(in) :: r0, r1, alp
real(8), intent(out) :: fdmp, dfdmp

fdmp = 1.0_8 / (1.0_8 + 6.0_8 * (r0 / r1)**(alp / 3.0_8))
dfdmp = -2.0_8 * alp * (r0 / r1)**(alp / 3.0_8) * fdmp**2
end subroutine

end program
