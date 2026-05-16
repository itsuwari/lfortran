module c_backend_inline_real_mod_01_m
contains
    real(8) function mod_r8(x, y)
        real(8), intent(in) :: x, y
        mod_r8 = mod(x, y)
    end function

    real function mod_r4(x, y)
        real, intent(in) :: x, y
        mod_r4 = mod(x, y)
    end function
end module

program c_backend_inline_real_mod_01
use c_backend_inline_real_mod_01_m, only: mod_r8, mod_r4
implicit none

if (abs(mod_r8(5.5_8, 2.0_8) - 1.5_8) > 1.0e-12_8) error stop
if (abs(mod_r8(-5.5_8, 2.0_8) + 1.5_8) > 1.0e-12_8) error stop
if (abs(mod_r8(5.5_8, -2.0_8) - 1.5_8) > 1.0e-12_8) error stop
if (abs(mod_r4(5.5, 2.0) - 1.5) > 1.0e-5) error stop
if (abs(mod_r4(-5.5, 2.0) + 1.5) > 1.0e-5) error stop

end program
