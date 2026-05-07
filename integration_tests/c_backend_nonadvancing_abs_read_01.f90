program c_backend_nonadvancing_abs_read_01
implicit none

type :: token_type
    integer :: first
    integer :: last
end type

integer, parameter :: bufsize = 512
character(len=*), parameter :: filename = "/tmp/04-val-xaj.gen"
character(len=*), parameter :: coord_record = &
    "    1    1    5.01008138495578E-01    8.68020297677281E-01    3.64847890045036E-01"
character(len=bufsize) :: msg
character(len=:), allocatable :: line
integer :: unit, stat
integer :: pos, lnum, dummy, isp
real(8) :: coord(3)
type(token_type) :: token

open(newunit=unit, file=filename, status="replace", action="write", iostat=stat)
if (stat /= 0) error stop 1
write(unit, "(a)", iostat=stat) "28 C"
if (stat /= 0) error stop 2
write(unit, "(a)", iostat=stat) " N C O H"
if (stat /= 0) error stop 3
write(unit, "(a)", iostat=stat) coord_record
if (stat /= 0) error stop 4
close(unit)

open(newunit=unit, file=filename, status="old", action="read", iostat=stat)
if (stat /= 0) error stop 5

pos = 0
lnum = 0
call advance_line(unit, line, pos, lnum, stat)
if (stat /= 0 .or. line /= "28 C") error stop 6
call advance_line(unit, line, pos, lnum, stat)
if (stat /= 0 .or. line /= "N C O H") error stop 7
call advance_line(unit, line, pos, lnum, stat)
if (stat /= 0) error stop 8
close(unit)

if (len(line) /= len_trim(adjustl(coord_record))) error stop 9
if (line /= trim(adjustl(coord_record))) error stop 10
pos = 0
call read_next_int(line, pos, token, dummy, stat)
if (stat /= 0 .or. dummy /= 1) error stop 11
call read_next_int(line, pos, token, isp, stat)
if (stat /= 0 .or. isp /= 1) error stop 12
call read_next_real(line, pos, token, coord(1), stat)
if (stat /= 0) error stop 13
call read_next_real(line, pos, token, coord(2), stat)
if (stat /= 0) error stop 14
call read_next_real(line, pos, token, coord(3), stat)
if (stat /= 0) error stop 15
if (abs(coord(3) - 3.64847890045036d-01) > 1.0d-14) error stop 16

contains

subroutine getline(unit, line, iostat)
    integer, intent(in) :: unit
    character(len=:), allocatable, intent(out) :: line
    integer, intent(out) :: iostat
    integer :: chunk
    character(len=bufsize) :: buffer

    line = ""
    do
        read(unit, "(a)", advance="no", iostat=iostat, iomsg=msg, size=chunk) buffer
        if (iostat > 0) exit
        line = line // buffer(:chunk)
        if (iostat < 0) then
            iostat = 0
            exit
        end if
    end do
end subroutine

subroutine next_line(unit, line, pos, lnum, iostat)
    integer, intent(in) :: unit
    character(len=:), allocatable, intent(out) :: line
    integer, intent(inout) :: pos
    integer, intent(inout) :: lnum
    integer, intent(out) :: iostat

    pos = 0
    call getline(unit, line, iostat)
    if (iostat == 0) lnum = lnum + 1
end subroutine

subroutine advance_line(unit, line, pos, lnum, iostat)
    integer, intent(in) :: unit
    character(len=:), allocatable, intent(out) :: line
    integer, intent(inout) :: pos
    integer, intent(inout) :: lnum
    integer, intent(out) :: iostat
    integer :: ihash

    iostat = 0
    do while (iostat == 0)
        call next_line(unit, line, pos, lnum, iostat)
        ihash = index(line, "#")
        if (ihash > 0) line = line(:ihash - 1)
        if (len_trim(line) > 0) exit
    end do
    line = trim(adjustl(line))
end subroutine

subroutine next_token(string, pos, token)
    character(len=*), intent(in) :: string
    integer, intent(inout) :: pos
    type(token_type), intent(out) :: token
    integer :: start

    if (pos >= len(string)) then
        token = token_type(len(string) + 1, len(string) + 1)
        return
    end if

    do while (pos < len(string))
        pos = pos + 1
        select case (string(pos:pos))
        case (" ", achar(9), achar(10), achar(13))
            continue
        case default
            exit
        end select
    end do

    start = pos
    do while (pos < len(string))
        pos = pos + 1
        select case (string(pos:pos))
        case (" ", achar(9), achar(10), achar(13))
            pos = pos - 1
            exit
        case default
            continue
        end select
    end do
    token = token_type(start, pos)
end subroutine

subroutine read_next_int(line, pos, token, val, iostat)
    character(len=*), intent(in) :: line
    integer, intent(inout) :: pos
    type(token_type), intent(inout) :: token
    integer, intent(out) :: val
    integer, intent(out) :: iostat

    call next_token(line, pos, token)
    if (token%first > 0 .and. token%last <= len(line)) then
        read(line(token%first:token%last), *, iostat=iostat) val
    else
        iostat = 1
    end if
end subroutine

subroutine read_next_real(line, pos, token, val, iostat)
    character(len=*), intent(in) :: line
    integer, intent(inout) :: pos
    type(token_type), intent(inout) :: token
    real(8), intent(out) :: val
    integer, intent(out) :: iostat

    call next_token(line, pos, token)
    if (token%first > 0 .and. token%last <= len(line)) then
        read(line(token%first:token%last), *, iostat=iostat) val
    else
        iostat = 1
    end if
end subroutine

end program
