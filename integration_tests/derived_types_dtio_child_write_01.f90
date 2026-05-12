module derived_types_dtio_child_write_01_m
    implicit none

    type :: t
    contains
        procedure, private :: write_t
        generic, public :: write(formatted) => write_t
    end type t

contains

    subroutine write_t(self, unit, iotype, v_list, iostat, iomsg)
        class(t), intent(in) :: self
        integer, intent(in) :: unit
        character(*), intent(in) :: iotype
        integer, intent(in) :: v_list(:)
        integer, intent(out) :: iostat
        character(*), intent(inout) :: iomsg

        iostat = 0
        write(unit, '(a)') 'AB'
        write(unit, '(a)') 'CD'
    end subroutine

end module derived_types_dtio_child_write_01_m

program derived_types_dtio_child_write_01
    use derived_types_dtio_child_write_01_m, only: t
    implicit none

    type(t) :: x
    character(len=100) :: line

    open(unit=20, file='_dtio_child_write_01.txt', status='replace')
    write(20, '(dt)') x
    close(20)

    open(unit=20, file='_dtio_child_write_01.txt', status='old')
    read(20, '(a)') line
    close(20, status='delete')

    if (trim(line) /= 'ABCD') error stop
end program derived_types_dtio_child_write_01
