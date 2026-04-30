program main
implicit none
real(8), allocatable :: w(:)
integer :: ixb

allocate(w(10))
w = 0.0d0
ixb = 4
w(ixb) = 3.0d0
call shift_three(w(ixb))
if (abs(w(ixb) - 4.0d0) > 1.0d-12) error stop
if (abs(w(ixb + 1) - 2.0d0) > 1.0d-12) error stop
if (abs(w(ixb + 2) - 3.0d0) > 1.0d-12) error stop

contains

subroutine shift_three(x)
    real(8), intent(inout) :: x(*)
    x(1) = x(1) + 1.0d0
    x(2) = x(2) + 2.0d0
    x(3) = x(3) + 3.0d0
end subroutine

end program
