/*
 * This code is part of QLLVM.
 *
 * (C) Copyright QCFlow 2026.
 *
 * This code is licensed under the Apache License, Version 2.0. You may
 * obtain a copy of this license in the LICENSE file in the root directory
 * of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Any modifications or derivative works of this code must retain this
 * copyright notice, and modified files need to carry a notice indicating
 * that they have been altered from the originals.
 */

/*******************************************************************************
 * Backend registry implementation
 *******************************************************************************/

 #include "qllvm/BackendRegistry.hpp"
 #include "qllvm/backends/QasmBackend.hpp"
 #include "qllvm/backends/TianyanBackend.hpp"
 #include "qllvm/backends/OriginQuantumBackend.hpp"
 #include "qllvm/backends/QirRunnerBackend.hpp"
 #include "qllvm/backends/IonQasmBackend.hpp"
 #include <algorithm>
 
 namespace qllvm {
 
 BackendRegistry& BackendRegistry::instance() {
   static BackendRegistry registry;
   return registry;
 }
 
 void BackendRegistry::registerBackend(std::unique_ptr<Backend> backend) {
   if (backend) {
     std::string n = backend->name();
     backends_[n] = std::move(backend);
   }
 }
 
 Backend* BackendRegistry::get(const std::string& name) const {
   auto it = backends_.find(name);
   return it != backends_.end() ? it->second.get() : nullptr;
 }
 
 std::vector<std::string> BackendRegistry::listBackends() const {
   std::vector<std::string> names;
   for (const auto& p : backends_)
     names.push_back(p.first);
   std::sort(names.begin(), names.end());
   return names;
 }
 
 /// Register built-in backends (called from registry init)
 static void registerBuiltinBackends() {
   BackendRegistry::instance().registerBackend(std::make_unique<QasmBackend>());
   BackendRegistry::instance().registerBackend(std::make_unique<TianyanBackend>());
   BackendRegistry::instance().registerBackend(std::make_unique<OriginQuantumBackend>());
   BackendRegistry::instance().registerBackend(std::make_unique<QirRunnerBackend>());
   BackendRegistry::instance().registerBackend(std::make_unique<IonQasmBackend>());
 }
 
 /// Static initializer to ensure backends are registered before first use
 struct BackendRegistrar {
   BackendRegistrar() { registerBuiltinBackends(); }
 };
 static BackendRegistrar s_registrar;
 
 }  // namespace qllvm
 