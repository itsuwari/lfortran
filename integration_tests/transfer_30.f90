program transfer_30
    use iso_fortran_env, only: int16, int32
    implicit none

    integer(int32), parameter :: zip_local_sig = int(z'04034b50', int32)
    integer(int16), parameter :: zip_min_version = 20_int16
    character(len=*), parameter :: filename = "transfer_30.bin"

    character(len=8) :: header
    integer :: stat, unit
    integer(int32) :: sig
    integer(int16) :: flags, version

    header = repeat("x", len(header))
    header(1:4) = transfer(zip_local_sig, header(1:4))
    header(5:6) = transfer(zip_min_version, header(5:6))
    header(7:8) = repeat(char(0), 2)

    open(newunit=unit, file=filename, status="replace", access="stream", &
        form="unformatted", action="write", iostat=stat)
    if (stat /= 0) error stop 1
    write(unit, iostat=stat) header
    if (stat /= 0) error stop 2
    close(unit)

    open(newunit=unit, file=filename, status="old", access="stream", &
        form="unformatted", action="read", iostat=stat)
    if (stat /= 0) error stop 3
    read(unit, iostat=stat) sig
    if (stat /= 0) error stop 4
    read(unit, iostat=stat) version
    if (stat /= 0) error stop 5
    read(unit, iostat=stat) flags
    if (stat /= 0) error stop 6
    close(unit)

    if (sig /= zip_local_sig) error stop 7
    if (version /= zip_min_version) error stop 8
    if (flags /= 0_int16) error stop 9
    if (iachar(header(7:7)) /= 0) error stop 10
    if (ichar(header(8:8)) /= 0) error stop 11
    if (shiftr(-1_int32, 8) /= int(z'00ffffff', int32)) error stop 12
end program
