module separate_compilation_50a
implicit none

type :: payload
    integer :: i
end type

contains

pure integer function iface_len()
    iface_len = 3
end function

integer function body_only()
    body_only = 4
end function

subroutine api(x, arr)
    type(payload), intent(in) :: x
    integer, intent(in) :: arr(iface_len())
    integer :: body_local

    body_local = body_only()
    if (body_local /= 4) error stop
    if (x%i /= 7) error stop
    if (sum(arr) /= 6) error stop
end subroutine

end module
