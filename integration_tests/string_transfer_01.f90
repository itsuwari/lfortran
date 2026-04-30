program string_transfer_01
    use iso_c_binding, only: c_char, c_null_char
    implicit none
    character(kind=c_char) :: buffer(6)

    call copy_to_c("abcd", buffer, size(buffer))

    if (buffer(1) /= "a") error stop
    if (buffer(2) /= "b") error stop
    if (buffer(3) /= "c") error stop
    if (buffer(4) /= "d") error stop
    if (buffer(5) /= c_null_char) error stop

contains

    subroutine copy_to_c(rhs, lhs, len)
        character(kind=c_char), intent(out) :: lhs(*)
        character(len=*), intent(in) :: rhs
        integer, intent(in) :: len
        integer :: length

        length = min(len - 1, len_trim(rhs))
        lhs(1:length) = transfer(rhs(1:length), lhs(1:length))
        lhs(length + 1:length + 1) = c_null_char
    end subroutine

end program
