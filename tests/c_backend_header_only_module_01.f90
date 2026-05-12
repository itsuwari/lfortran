module c_backend_header_only_module_01
implicit none

type :: payload_t
    integer :: value
end type

interface
    subroutine declared_only(x)
        import :: payload_t
        type(payload_t), intent(in) :: x
    end subroutine
end interface

end module
