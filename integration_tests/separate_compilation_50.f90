program separate_compilation_50
    use separate_compilation_50a, only: payload, api
    implicit none

    type(payload) :: x
    integer :: arr(3)

    x%i = 7
    arr = [1, 2, 3]
    call api(x, arr)
end program
