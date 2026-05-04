program array_section_31
    implicit none

    type map_t
        integer :: idx(2)
    end type

    type(map_t) :: map
    real(8) :: pao(3), vec(3), mpat(3, 2, 2), shifted(5)
    integer :: iao, spin, i

    map%idx = [2, 1]
    pao = [1.0d0, 2.0d0, 3.0d0]
    vec = [10.0d0, 20.0d0, 30.0d0]

    pao(:) = pao + 2.0d0 * vec(:)

    if (abs(pao(1) - 21.0d0) > 1.0d-12) error stop
    if (abs(pao(2) - 42.0d0) > 1.0d-12) error stop
    if (abs(pao(3) - 63.0d0) > 1.0d-12) error stop

    mpat = 0.0d0
    iao = 1
    spin = 2
    do i = 1, 3
        mpat(i, map%idx(iao), spin) = 4.0d0 + i
    end do
    mpat(:, map%idx(iao), spin) = mpat(:, map%idx(iao), spin) - pao

    if (abs(mpat(1, 2, 2) + 16.0d0) > 1.0d-12) error stop
    if (abs(mpat(2, 2, 2) + 36.0d0) > 1.0d-12) error stop
    if (abs(mpat(3, 2, 2) + 56.0d0) > 1.0d-12) error stop

    shifted = [1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0]
    shifted(2:5) = shifted(1:4)

    if (abs(shifted(1) - 1.0d0) > 1.0d-12) error stop
    if (abs(shifted(2) - 1.0d0) > 1.0d-12) error stop
    if (abs(shifted(3) - 2.0d0) > 1.0d-12) error stop
    if (abs(shifted(4) - 3.0d0) > 1.0d-12) error stop
    if (abs(shifted(5) - 4.0d0) > 1.0d-12) error stop
end program
