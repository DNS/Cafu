/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "AllComponents.hpp"
#include "HumanPlayer/CompCarriedWeapon.hpp"
#include "HumanPlayer/CompInventory.hpp"
#include "CompBase.hpp"
#include "CompBasics.hpp"
#include "CompCollisionModel.hpp"
#include "CompHumanPlayer.hpp"
#include "CompLight.hpp"
#include "CompLightPoint.hpp"
#include "CompLightRadiosity.hpp"
#include "CompModel.hpp"
#include "CompMover.hpp"
#include "CompParticleSystemOld.hpp"
#include "CompPhysics.hpp"
#include "CompPlayerPhysics.hpp"
#include "CompPlayerStart.hpp"
#include "CompScript.hpp"
#include "CompSound.hpp"
#include "CompTarget.hpp"
#include "CompTransform.hpp"


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& cf::GameSys::GetComponentTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


bool cf::GameSys::IsFundamental(const cf::TypeSys::TypeInfoT* CompType)
{
    if (CompType == &ComponentBasicsT::TypeInfo) return true;
    if (CompType == &ComponentTransformT::TypeInfo) return true;

    return false;
}


/*
 * Intentionally use a named, non-anonymous namespace here to give AllComponentTIs[] external linkage.
 * With an anonymous namespace, the compiler might optimize AllComponentTIs[] out, defeating its purpose.
 *
 * The purpose of this array is to make sure that the constructors of all static TypeInfoT
 * members of all ComponentBaseT derived classes have been run and thus the TypeInfoTs all
 * registered themselves at the global type info manager (TypeInfoManT).
 *
 * Q: Why isn't that automatically the case, given that all TypeInfoTs are *static* members of
 *    their classes and supposed to be initialized before main() begins anyway?
 *
 * First, the C++ standard does not guarantee that nonlocal objects with static storage duration
 * are initialized before main() begins. Rather, their initialization can be deferred until
 * before their first use. This problem would be fixed by calling this function early in main(),
 * but as has been convincingly explained by James Kanze in [1], that is not an issue anyway:
 * Compilers just do not implement deferred initialization, mostly for backward-compatibility.
 *
 * The second and more important factor is the linker:
 * Both under Windows and Linux (and probably everywhere else), linkers include the symbols in
 * static libraries only in the executables if they resolve an unresolved external. This is
 * contrary to .obj files that are given to the linker directly [1].
 *
 * Thus, with the GameSys files all being part of a library, there are only two approaches to
 * make sure that all relevant units make it into the executable: Either pass the object files
 * directly and individually to the linker, or employ an array like our AllComponentTIs.
 *
 * The problem with passing the individual object files is that this is difficult to implement
 * in SCons, and probably any other build system. Therefore, the only method for solving the
 * problem a reliable and portable manner that works well with any build system seems to be
 * the use of a method like AllComponentTIs.
 *
 * [1] For more details, see the thread "Can initialization of static class members be forced
 *     before main?" that I've begun on 2008-Apr-03 in comp.lang.c++:
 *     http://groups.google.de/group/comp.lang.c++/browse_thread/thread/e264caa531ff52a9/
 *     Another very good explanation is at:
 *     http://blog.copton.net/articles/linker/index.html#linker-dependencies
 */
namespace cf
{
    namespace GameSys
    {
        const cf::TypeSys::TypeInfoT* AllComponentTIs[] =
        {
            &ComponentBaseT::TypeInfo,
            &ComponentBasicsT::TypeInfo,
            &ComponentCarriedWeaponT::TypeInfo,
            &ComponentCollisionModelT::TypeInfo,
            &ComponentHumanPlayerT::TypeInfo,
            &ComponentInventoryT::TypeInfo,
            &ComponentLightT::TypeInfo,
            &ComponentModelT::TypeInfo,
            &ComponentMoverT::TypeInfo,
            &ComponentParticleSystemOldT::TypeInfo,
            &ComponentPhysicsT::TypeInfo,
            &ComponentPlayerPhysicsT::TypeInfo,
            &ComponentPlayerStartT::TypeInfo,
            &ComponentPointLightT::TypeInfo,
            &ComponentRadiosityLightT::TypeInfo,
            &ComponentScriptT::TypeInfo,
            &ComponentSoundT::TypeInfo,
            &ComponentTargetT::TypeInfo,
            &ComponentTransformT::TypeInfo,
        };
    }
}
