#include "sema/emitter.h"

namespace mslc {

// A struct of thread pointers to textures and samplers, and of such structs (ANGLE's TextureEnv and the struct
// of them it passes to every function). It is not a value, since mslc has no run-time texture values: a local of
// it is a set of names for descriptors, each field bound by an assignment, and a function taking it is emitted
// once per set of descriptors it is given. Null for any other type; a struct that mixes such a field with
// others is refused.
const StructDecl* Emitter::resourceStructOf(const Type& type, unsigned depth) {
	if (type.namedType.empty() || type.isPointer || type.arrayLength || type.resource != ResourceKind::None) {
		return nullptr;
	}
	const StructDecl* decl = _unit.findStruct(type.namedType);
	if (!decl || decl->fields.empty() || depth > 16) {
		return nullptr;
	}
	bool resource = false;
	bool other = false;
	for (const StructField& field: decl->fields) {
		const bool leaf = field.type.resource != ResourceKind::None && field.type.isPointer;
		(leaf || resourceStructOf(field.type, depth + 1) ? resource : other) = true;
	}
	if (resource && other) {
		throw CompileError("the struct \"" + decl->name + "\" has texture or sampler pointer members beside other "
			"members, which is not lowered; mslc lowers a struct whose members are all texture and sampler "
			"pointers (or such structs)");
	}
	return resource ? decl : nullptr;
}

// The binding of a resource struct an expression names: a local or parameter of one, or a member of one that
// is one itself (created when first named, so that its fields can be assigned).
Binding* Emitter::resourceAggregate(const Expression& expression) {
	if (expression.kind == ExpressionKind::Identifier) {
		const auto found = _bindings.find(expression.name);
		return !expression.fileScope && found != _bindings.end() && found->second.resourceStruct
			? &found->second : nullptr;
	}
	if (expression.kind != ExpressionKind::Member || expression.isArrow || !expression.left) {
		return nullptr;
	}
	Binding* parent = resourceAggregate(*expression.left);
	if (!parent) {
		return nullptr;
	}
	const StructField* field = nullptr;
	for (const StructField& candidate: parent->resourceStruct->fields) {
		if (candidate.name == expression.memberName) {
			field = &candidate;
		}
	}
	if (!field) {
		throw CompileError("\"" + parent->resourceStruct->name + "\" has no member \"" + expression.memberName + "\"");
	}
	const StructDecl* nested = resourceStructOf(field->type);
	if (!nested) {
		return nullptr;
	}
	auto [slot, inserted] = parent->resourceFields->try_emplace(field->name);
	if (inserted) {
		slot->second.resourceStruct = nested;
		slot->second.resourceFields = std::make_shared<std::map<std::string, Binding>>();
		slot->second.resourceDepth = parent->resourceDepth;
		slot->second.resourceParameter = parent->resourceParameter;
	}
	return &slot->second;
}

// A copy of a resource struct's names, which assigning the copy's members does not change; its members are
// assigned where the copy is declared.
static Binding copyResourceStruct(const Binding& from, uint32_t depth) {
	Binding copy = from;
	copy.resourceParameter = false;
	copy.resourceDepth = depth;
	copy.resourceFields = std::make_shared<std::map<std::string, Binding>>(*from.resourceFields);
	for (auto& entry: *copy.resourceFields) {
		if (entry.second.resourceStruct) {
			entry.second = copyResourceStruct(entry.second, depth);
		}
	}
	return copy;
}

// "S s;", whose members are then assigned, or "S s = other;", a copy of another's.
void Emitter::declareResourceStruct(const VariableDeclaration& declaration, const StructDecl& decl) {
	const Binding* source = declaration.initializer ? resourceAggregate(*declaration.initializer) : nullptr;
	if ((declaration.initializer && (!source || source->resourceStruct != &decl)) || declaration.isReference) {
		throw CompileError("the " + decl.name + " \"" + declaration.name + "\" is a struct of texture and sampler "
			"pointers, which mslc lowers as a local declared without an initialiser whose members are then "
			"assigned, or as a copy of another " + decl.name);
	}
	Binding binding;
	if (source) {
		binding = copyResourceStruct(*source, _controlDepth);
	} else {
		binding.resourceStruct = &decl;
		binding.resourceFields = std::make_shared<std::map<std::string, Binding>>();
	}
	binding.resourceDepth = _controlDepth;
	_bindings[declaration.name] = binding;
}

// "s.texture = &t;" for a resource struct s: binds the member to the descriptor, once, where s was declared (not
// under a condition or in a loop it encloses). False for any other assignment.
bool Emitter::assignResourceField(const Expression& left, const Expression& right, bool compound) {
	if (left.kind != ExpressionKind::Member || left.isArrow || !left.left) {
		return false;
	}
	Binding* aggregate = resourceAggregate(*left.left);
	if (!aggregate) {
		return false;
	}
	const StructField* field = nullptr;
	for (const StructField& candidate: aggregate->resourceStruct->fields) {
		if (candidate.name == left.memberName) {
			field = &candidate;
		}
	}
	const std::string what = "the member \"" + left.memberName + "\" of a " + aggregate->resourceStruct->name;
	if (!field) {
		throw CompileError("\"" + aggregate->resourceStruct->name + "\" has no member \"" + left.memberName + "\"");
	}
	if (resourceStructOf(field->type)) {
		throw CompileError(what + " is a struct of texture and sampler pointers, which is assigned member by "
			"member");
	}
	if (compound) {
		throw CompileError(what + " is a texture or sampler pointer, which a compound assignment does not take");
	}
	if (aggregate->resourceParameter) {
		throw CompileError(what + " is a parameter's, which mslc does not assign: the descriptor it names is "
			"the caller's");
	}
	if (_controlDepth != aggregate->resourceDepth) {
		throw CompileError(what + " is assigned under a condition or in a loop, which would choose the "
			"descriptor it names at run time; mslc lowers an assignment where the struct is declared");
	}
	if (aggregate->resourceFields->count(field->name)) {
		throw CompileError(what + " is assigned again; mslc binds each member of a struct of texture and "
			"sampler pointers once");
	}
	const std::optional<ResolvedResource> resolved = resourceOf(right);
	if (!resolved || !resolved->pointer || !sameResourceType(resolved->binding.pointeeMsl, field->type)) {
		throw CompileError(what + " is a " + typeName(field->type) + " pointer, and is assigned something that "
			"is not the address of one");
	}
	if (resolved->binding.embeddedSampler && !field->type.isConst) {
		throw CompileError(what + " is assigned the address of a constexpr sampler, which is const; the member "
			"has to be a pointer to const");
	}
	Binding leaf = resolved->binding;
	leaf.resourcePointer = true;
	(*aggregate->resourceFields)[field->name] = leaf;
	return true;
}

// The descriptor ids a resource struct names, in member order, each member assigned and each the module's own
// variable (a helper's texture parameter is that helper's, so no other function can be specialised on it).
void Emitter::resourceStructLeaves(const Binding& aggregate, const std::string& path, std::vector<Id>& ids,
	const std::string& what) {
	for (const StructField& field: aggregate.resourceStruct->fields) {
		const std::string named = path + "." + field.name;
		const auto found = aggregate.resourceFields->find(field.name);
		if (resourceStructOf(field.type)) {
			if (found == aggregate.resourceFields->end()) {
				throw CompileError(what + ": its member \"" + named.substr(1) + "\" is never assigned");
			}
			resourceStructLeaves(found->second, named, ids, what);
			continue;
		}
		if (found == aggregate.resourceFields->end()) {
			throw CompileError(what + ": its member \"" + named.substr(1) + "\" is not assigned before the call");
		}
		if (found->second.functionResource) {
			throw CompileError(what + ": its member \"" + named.substr(1) + "\" names a texture or sampler "
				"parameter of the calling helper, and mslc passes a struct of texture and sampler pointers "
				"built from the entry point's own only");
		}
		ids.push_back(found->second.id);
	}
}

} // namespace mslc
