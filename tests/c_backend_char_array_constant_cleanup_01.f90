program c_backend_char_array_constant_cleanup_01
implicit none

if (.not. is_digit("5")) error stop
if (is_digit("x")) error stop

contains

logical function is_digit(ch)
    character(len=*), intent(in) :: ch
    character(len=1), parameter :: digits(10) = &
        ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"]
    integer :: i

    is_digit = .false.
    do i = 1, 10
        if (digits(i) == ch) then
            is_digit = .true.
            return
        end if
    end do
end function

end program
