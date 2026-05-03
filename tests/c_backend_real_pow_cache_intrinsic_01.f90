subroutine c_backend_real_pow_cache_intrinsic_01(rc, norm_exp, kcn, r, count)
implicit none

real(8), intent(in) :: rc, norm_exp, kcn, r
real(8), intent(out) :: count

count = 0.5_8 * (1.0_8 + erf(-kcn*(r - rc)/rc**norm_exp))

end subroutine
