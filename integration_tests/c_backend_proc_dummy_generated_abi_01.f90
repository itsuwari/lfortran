module c_backend_proc_dummy_generated_abi_01_mod
implicit none

abstract interface
    pure function binary_real_op(x, y) result(r)
        real(8), intent(in) :: x, y
        real(8) :: r
    end function
end interface

contains

pure function apply_op(op, x, y) result(r)
    procedure(binary_real_op) :: op
    real(8), intent(in) :: x, y
    real(8) :: r

    r = op(2.0d0*x, y)
end function

pure function add_real(x, y) result(r)
    real(8), intent(in) :: x, y
    real(8) :: r

    r = x + y
end function

end module

program c_backend_proc_dummy_generated_abi_01
use c_backend_proc_dummy_generated_abi_01_mod, only: apply_op, add_real
implicit none

real(8) :: value

value = apply_op(add_real, 3.0d0, 4.0d0)
if (abs(value - 10.0d0) > 1.0d-12) error stop

end program
