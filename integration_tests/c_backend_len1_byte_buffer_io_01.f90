program c_backend_len1_byte_buffer_io_01
implicit none

character(len=1), allocatable :: written(:), read_back(:)
integer :: unit, i
integer, parameter :: n = 257

allocate(written(n), read_back(n))
written = achar(0)
read_back = achar(99)

do i = 1, n
    written(i) = achar(mod(i - 1, 256))
end do

open(newunit=unit, file="c_backend_len1_byte_buffer_io_01.bin", &
    access="stream", form="unformatted", status="replace", action="write")
write(unit) written
close(unit)

open(newunit=unit, file="c_backend_len1_byte_buffer_io_01.bin", &
    access="stream", form="unformatted", status="old", action="read")
read(unit) read_back
close(unit, status="delete")

do i = 1, n
    if (iachar(read_back(i)) /= mod(i - 1, 256)) error stop "byte roundtrip mismatch"
end do

deallocate(written, read_back)

end program c_backend_len1_byte_buffer_io_01
