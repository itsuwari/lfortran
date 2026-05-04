program c_backend_allocatable_string_cleanup_01
implicit none

call check_cleanup()

contains

subroutine check_cleanup()
integer :: i
character(len=8) :: buffer
character(len=:), allocatable :: label

buffer = "ab"
if (trim(buffer) /= "ab") error stop

do i = 1, 4
    label = make_label(i)
    if (label /= "ab") error stop
    if (make_label(i) /= "ab") error stop
end do

end subroutine

function make_label(i) result(label)
    integer, intent(in) :: i
    character(len=:), allocatable :: label
    character(len=:), allocatable :: tmp

    if (i == 1) then
        tmp = trim("ab  ")
    else
        tmp = "a" // "b"
    end if
    label = tmp
end function

end program
