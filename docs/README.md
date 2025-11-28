# MICA Ecosystem - Documentation

Complete documentation for the MICA IoT devices ecosystem.

## 📚 Documentation Index

### Core Documentation

#### [architecture.md](./architecture.md)
Complete system architecture for the MICA Recirculator device:
- Layered architecture pattern (Application → Services → Drivers)
- Module descriptions and responsibilities
- FreeRTOS task organization
- MQTT topics and communication protocols
- Hardware interfaces and pinout

**Read this first** to understand the system design and module interactions.

---

#### [hardware.md](./hardware.md)
Hardware specifications and wiring diagrams:
- ESP32-C3 pinout and connections
- Component list (relay, temperature sensor, OLED, LED, buttons)
- Power requirements
- Physical connections and schematic

Reference this when working with hardware or troubleshooting physical connections.

---

### Development & Planning

#### [ISSUES.md](./ISSUES.md)
Active development tasks and issue tracking:
- Open issues with priority levels
- Completed tasks history
- Bug reports and feature requests
- Implementation notes and decisions

Check here for current work items and project status.

---

#### [REFACTORING-PLAN.md](./REFACTORING-PLAN.md)
Strategic refactoring initiatives:
- Monorepo migration strategy
- Code organization improvements
- Module consolidation plans
- Breaking changes and migration paths

Review before making architectural changes.

---

#### [CHANGELOG-2025-11-28.md](./CHANGELOG-2025-11-28.md)
Detailed change history for major refactoring session:
- Implemented features
- Bug fixes
- Refactoring decisions
- Migration notes

Historical reference for understanding recent changes.

---

## 🎯 Quick Navigation

**New to the project?** → Start with [architecture.md](./architecture.md)

**Setting up hardware?** → Check [hardware.md](./hardware.md)

**Looking for tasks?** → See [ISSUES.md](./ISSUES.md)

**Planning changes?** → Review [REFACTORING-PLAN.md](./REFACTORING-PLAN.md)

**Need history?** → Browse [CHANGELOG-2025-11-28.md](./CHANGELOG-2025-11-28.md)

---

## 🏗️ Project Structure Reference

```
mica-ecosystem/
├── apps/
│   └── recirculator/         # Water pump control device
│       ├── platformio.ini    # PlatformIO configuration
│       └── src/              # Source code
├── libs/
│   └── core/                 # Shared modules (planned)
├── docs/                     # 📍 You are here
└── .github/
    └── copilot-instructions.md  # Development guidelines
```

---

## 🛠️ Development Guidelines

For coding standards, workflow, and best practices, see:
- [/.github/copilot-instructions.md](../.github/copilot-instructions.md)

This includes:
- Code style and naming conventions
- Git workflow and commit guidelines
- Testing procedures
- Architecture principles by gaesca04

---

**Last Updated**: 28 November 2025  
**Architecture by**: gaesca04 (Computer Engineer)
