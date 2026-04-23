module global_array_02_mod
implicit none

integer, parameter :: max_elem = 4
integer, dimension(max_elem) :: refn

contains

function get_nref(num) result(n)
    integer, intent(in) :: num
    integer :: n

    if (num > 0 .and. num <= size(refn)) then
        n = num
    else
        n = 0
    end if
end function

subroutine set_ref(i, value)
    integer, intent(in) :: i, value

    refn(i) = value
end subroutine

function get_ref(i) result(value)
    integer, intent(in) :: i
    integer :: value

    value = refn(i)
end function

end module

program global_array_02
use global_array_02_mod
implicit none

call set_ref(2, 7)
if (get_nref(4) /= 4) error stop "size failed"
if (get_nref(5) /= 0) error stop "bounds failed"
if (get_ref(2) /= 7) error stop "data failed"

end program
