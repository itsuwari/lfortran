module derived_type_with_default_init_02_m
    implicit none

    type :: logger_base
    contains
        procedure :: message
    end type

    type :: context_type
        integer :: unit = 6
        class(logger_base), allocatable :: io
    contains
        procedure :: write_message
    end type

contains

    subroutine message(self, text)
        class(logger_base), intent(inout) :: self
        character(len=*), intent(in) :: text

        if (len(text) == 0) error stop "empty"
    end subroutine

    subroutine write_message(self, text)
        class(context_type), intent(inout) :: self
        character(len=*), intent(in) :: text

        if (self%unit /= 6) error stop "default scalar init failed"
        if (allocated(self%io)) then
            call self%io%message(text)
        end if
    end subroutine

    subroutine check()
        type(context_type) :: ctx

        call ctx%write_message("ok")
    end subroutine

end module

program derived_type_with_default_init_02
    use derived_type_with_default_init_02_m, only: check
    implicit none

    call check()
end program
