program c_backend_function_result_intent_out_struct_01
    implicit none

    type :: field_type
        character(len=:), allocatable :: label
        real(8) :: values(3)
    end type

    type(field_type) :: field
    real(8) :: values(3)

    values = [1.0d0, 2.0d0, 3.0d0]
    field = make_field(values)

    if (.not. allocated(field%label)) error stop
    if (field%label /= "field") error stop
    if (any(field%values /= values)) error stop

contains

    function make_field(values) result(new)
        real(8), intent(in) :: values(:)
        type(field_type) :: new

        call init_field(new, values)
    end function

    subroutine init_field(self, values)
        type(field_type), intent(out) :: self
        real(8), intent(in) :: values(:)

        self%label = "field"
        self%values(:) = values(:3)
    end subroutine
end program
