program c_backend_allocatable_string_cleanup_01
implicit none

integer :: i

do i = 1, 4
    if (make_label(i) /= "ab") error stop
end do

contains

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
