module c_backend_reachable_enum_member_01
implicit none
private
public :: enum_member_value

enum, bind(c)
    enumerator :: enum_member_marker_314159 = 17
end enum

contains

integer function enum_member_value()
enum_member_value = enum_member_marker_314159
end function

end module
