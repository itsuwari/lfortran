module c_backend_inline_integer_modulo_01_m
contains
    integer function parity(x)
        integer, intent(in) :: x
        parity = modulo(x, 2)
    end function

    integer function negative_parity(x)
        integer, intent(in) :: x
        negative_parity = modulo(x, -2)
    end function

    integer function signed_remainder(x)
        integer, intent(in) :: x
        signed_remainder = mod(x, 2)
    end function
end module

program c_backend_inline_integer_modulo_01
use c_backend_inline_integer_modulo_01_m, only: parity, negative_parity, signed_remainder
implicit none

integer :: vals(7)

vals = [parity(-3), parity(-2), parity(-1), parity(0), parity(1), parity(2), parity(3)]
if (any(vals /= [1, 0, 1, 0, 1, 0, 1])) error stop

vals = [negative_parity(-3), negative_parity(-2), negative_parity(-1), &
        negative_parity(0), negative_parity(1), negative_parity(2), negative_parity(3)]
if (any(vals /= [-1, 0, -1, 0, -1, 0, -1])) error stop

if (signed_remainder(-3) /= -1) error stop
if (signed_remainder(3) /= 1) error stop

end program
