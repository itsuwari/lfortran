module separate_compilation_50a
use separate_compilation_50dep, only: payload
implicit none
private
public :: payload, api

contains

pure integer function iface_len()
    iface_len = 3
end function

integer function body_only()
    body_only = 4
end function

subroutine api(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    call worker(x, arr)
end subroutine

subroutine worker(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len(), x%i)
    integer :: body_local

    body_local = body_only()
    if (body_local /= 4) error stop
    if (x%i /= 7) error stop
    if (sum(arr) /= 168) error stop
end subroutine

end module
