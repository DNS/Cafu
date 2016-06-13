/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_MANAGER_HPP_INCLUDED
#define CAFU_MODEL_MANAGER_HPP_INCLUDED

#include <map>
#include <string>


class CafuModelT;


/// This class is used for managing model instances.
///
/// Without this class, when several users each create their own model instance from the same filename,
/// the model is physically instantiated multiple times. Each instance consumes resources (such as the
/// allocated memory), but is exactly identical to the others.
/// However, once a model has been created, it is \emph{immutable} (const), because all its variable
/// state is extrinsic, i.e. seperately kept in \c AnimPoseT instances.
///
/// Therefore, very much as in the Flyweight pattern, model instances can be shared among users:
/// It suffices to physically instantiate each model only once and to point users to that instance.
///
/// In summary, the purpose of this class is to share model instances among users in order to avoid
/// resource duplication. It also calls the proper loaders according to the requested model filename,
/// and gracefully handles errors by substituting a dummy model instead of propagating an exception.
class ModelManagerT
{
    public:

    ModelManagerT();
    ~ModelManagerT();

    /// Returns (a pointer to) a model instance for the given filename.
    /// The returned model instance is possibly shared with other users, and must not be deleted.
    ///
    /// If there was an error loading the model, a "dummy model" instance is returned
    /// (no exception is thrown and the return value is a valid pointer to the dummy model).
    /// When this is the first attempt to load the model, an error message is returned as well;
    /// further requests to the same model will just return the dummy model but not the message.
    ///
    /// @param FileName   The filename to load the model from.
    /// @param ErrorMsg   Is set to the error message if there was an error loading the model,
    ///                   or the empty string \c "" when the model was successfully loaded.
    const CafuModelT* GetModel(const std::string& FileName, std::string* ErrorMsg=NULL) const;


    private:

    ModelManagerT(const ModelManagerT&);        ///< Use of the Copy Constructor    is not allowed.
    void operator = (const ModelManagerT&);     ///< Use of the Assignment Operator is not allowed.

    mutable std::map<std::string, CafuModelT*> m_Models;
};

#endif
