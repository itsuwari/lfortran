program c_backend_transfer_char_reshape_array_01
use iso_fortran_env, only: int64, real64
implicit none

integer(int64) :: int2(2, 3), expect_int2(2, 3)
integer(int64) :: int3(2, 3, 2), expect_int3(2, 3, 2)
real(real64) :: out2(2, 3)
real(real64) :: out3(2, 3, 2)
character(len=1) :: ibytes2(6 * 8)
character(len=1) :: ibytes3(12 * 8)
character(len=1) :: bytes2(6 * 8)
character(len=1) :: bytes3(12 * 8)
character(len=1) :: small(2)
character(len=1) :: scalar_bytes(8)
integer(int64) :: scalar_value
integer :: pos

call fill_i64_low_byte(ibytes2, size(int2))
call fill_i64_low_byte(ibytes3, size(int3))
bytes2 = achar(0)
bytes3 = achar(0)
small = achar(0)
small(1) = achar(65)
if (iachar(small(1)) /= 65) error stop "wrong small scalar character fill"
if (iachar(small(2)) /= 0) error stop "wrong small nul character fill"

scalar_value = 1_int64
scalar_bytes = transfer(scalar_value, scalar_bytes)
if (iachar(scalar_bytes(1)) /= 1) error stop "wrong scalar transfer low byte"
do pos = 2, size(scalar_bytes)
    if (iachar(scalar_bytes(pos)) /= 0) error stop "wrong scalar transfer high byte"
end do

expect_int2 = reshape([1_int64, 2_int64, 3_int64, 4_int64, 5_int64, 6_int64], shape(int2))
expect_int3 = reshape([1_int64, 2_int64, 3_int64, 4_int64, 5_int64, 6_int64, &
    7_int64, 8_int64, 9_int64, 10_int64, 11_int64, 12_int64], shape(int3))
int2 = -999_int64
int3 = -999_int64
pos = 0
call read_i2(ibytes2, pos, int2)
if (pos /= size(int2) * 8) error stop "wrong integer rank-2 byte position"
if (any(int2 /= expect_int2)) error stop "wrong integer rank-2 transfer"

pos = 0
call read_i3(ibytes3, pos, int3)
if (pos /= size(int3) * 8) error stop "wrong integer rank-3 byte position"
if (any(int3 /= expect_int3)) error stop "wrong integer rank-3 transfer"

out2 = -999.0_real64
out3 = -999.0_real64
pos = 0
call read_r2(bytes2, pos, out2)
if (pos /= size(out2) * 8) error stop "wrong rank-2 byte position"
if (any(out2 /= 0.0_real64)) error stop "wrong rank-2 transfer"

pos = 0
call read_r3(bytes3, pos, out3)
if (pos /= size(out3) * 8) error stop "wrong rank-3 byte position"
if (any(out3 /= 0.0_real64)) error stop "wrong rank-3 transfer"

contains

    subroutine fill_i64_low_byte(buffer, nelem)
    character(len=1), intent(out) :: buffer(:)
    integer, intent(in) :: nelem
    integer :: i, base

    buffer = achar(0)
    do i = 1, nelem
        base = (i - 1) * 8
        buffer(base + 1) = achar(i)
    end do
    end subroutine fill_i64_low_byte

    subroutine read_i2(buffer, pos, var)
    character(len=1), intent(in) :: buffer(:)
    integer, intent(inout) :: pos
    integer(int64), intent(out) :: var(:, :)

    var = reshape(transfer(buffer(pos + 1:pos + size(var) * 8), var), shape(var))
    pos = pos + size(var) * 8
    end subroutine read_i2

    subroutine read_i3(buffer, pos, var)
    character(len=1), intent(in) :: buffer(:)
    integer, intent(inout) :: pos
    integer(int64), intent(out) :: var(:, :, :)

    var = reshape(transfer(buffer(pos + 1:pos + size(var) * 8), var), shape(var))
    pos = pos + size(var) * 8
    end subroutine read_i3

    subroutine read_r2(buffer, pos, var)
    character(len=1), intent(in) :: buffer(:)
    integer, intent(inout) :: pos
    real(real64), intent(out) :: var(:, :)

    var = reshape(transfer(buffer(pos + 1:pos + size(var) * 8), var), shape(var))
    pos = pos + size(var) * 8
    end subroutine read_r2

    subroutine read_r3(buffer, pos, var)
    character(len=1), intent(in) :: buffer(:)
    integer, intent(inout) :: pos
    real(real64), intent(out) :: var(:, :, :)

    var = reshape(transfer(buffer(pos + 1:pos + size(var) * 8), var), shape(var))
    pos = pos + size(var) * 8
    end subroutine read_r3

end program c_backend_transfer_char_reshape_array_01
