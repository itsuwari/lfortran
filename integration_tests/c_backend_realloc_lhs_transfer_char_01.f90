program c_backend_realloc_lhs_transfer_char_01
    implicit none

    character(len=1) :: chunk(4)
    integer :: value

    value = int(z'01020304')
    chunk = transfer(value, chunk)

    if (size(chunk) /= 4) error stop
    if (iachar(chunk(1)) + iachar(chunk(2)) + iachar(chunk(3)) + iachar(chunk(4)) <= 0) error stop
end program
