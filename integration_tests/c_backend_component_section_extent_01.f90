module c_backend_component_section_extent_01_m
implicit none

type :: int_box
    integer, allocatable :: values(:)
end type

contains

subroutine consume_section(section, observed_size, observed_sum)
    integer, intent(in) :: section(:)
    integer, intent(out) :: observed_size, observed_sum
    integer :: i

    observed_size = size(section)
    observed_sum = 0
    do i = 1, size(section)
        observed_sum = observed_sum + section(i)
    end do
end subroutine

subroutine call_component_section(box, first, count, observed_size, observed_sum)
    type(int_box), intent(in) :: box
    integer, intent(in) :: first, count
    integer, intent(out) :: observed_size, observed_sum

    call consume_section(box%values(first:first+count-1), observed_size, observed_sum)
end subroutine

end module

program c_backend_component_section_extent_01
use c_backend_component_section_extent_01_m, only: int_box, call_component_section
implicit none

type(int_box) :: box
integer :: observed_size, observed_sum

allocate(box%values(8))
box%values = [1, 2, 3, 4, 1000, 2000, 4000, 8000]

call call_component_section(box, 2, 3, observed_size, observed_sum)

if (observed_size /= 3) error stop
if (observed_sum /= 9) error stop

end program
