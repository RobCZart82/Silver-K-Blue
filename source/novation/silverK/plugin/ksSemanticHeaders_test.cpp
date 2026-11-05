// Compile from outside ksNative: quoted local headers in the production guard
// cannot detect an incorrect public include root exported to consumers.
#include "ksLib/ksBoard.h"
#include <type_traits>
#include <utility>

static_assert(std::is_same_v<decltype(std::declval<const ks::Board&>().nativeOwnerState()),
                             const ks::BoardNativeOwner*>,
              "Semantic consumers must receive the native Board definition");
static_assert(std::is_same_v<decltype(std::declval<const ks::Board&>().h8InterpretedSteps()),
                             uint64_t>);
