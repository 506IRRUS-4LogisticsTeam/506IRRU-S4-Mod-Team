# 506th IRRU — S-4 Mod Team Source

Source code repository for all custom Arma Reforger mods developed by the 506th IRRU S-4 Mod Team.

**Team site, roadmap, and mod catalog:** [https://506irrus-4logisticsteam.github.io/team-info/](https://506irrus-4logisticsteam.github.io/team-info/)

---

## Repository Structure

Each top-level folder is an individual mod project containing scripts, prefabs, configs, and assets for the Arma Reforger Workbench.

| Folder | Mod |
|--------|-----|
| `506IRRUCore` | Core utility framework |
| `506IRRU All in One` | Meta-mod — bundles all 506th mods |
| `506IRRUFactions` | Faction definitions and configs |
| `506IRRUConflict` | Conflict scenario addon |
| `506IRRUMissionTemplates` | Mission and world templates |
| `506IRRU Medical` | Medical system (CPR, casualty inspection) |
| `506IRRU Respiratory` | Pneumothorax / respiratory injuries |
| `506IRRU Loadouts` | Loadout save/load system |
| `506IRRU - LoadoutManager` | Standalone Arsenal loadout manager |
| `506IRRUEquipment` | Equipment configs (lasers, magnifiers) |
| `506IRRUWeapons` | Weapon configs and modifications |
| `506IRRUVehicles` | Vehicle configurations |
| `506IRRU A10C` | A-10C Warthog HUD components |
| `506IRRU Airplane Props` | Decorative aircraft assets |
| `506thIRRUEnhancedRadio` | Enhanced VON / radio system |
| `506thIRRUCustomNameTags` | Player name tags |
| `506IRRU Ticket System` | In-game support ticket system |
| `506IRRU GM Contact View` | GM contact view with color coding |
| `506thIRRUMortars` | Mortar artillery system |
| `506IRRU - Add Forces Ares Slot` | ForceGM group slots |

---

## Loadout Manager

The standalone Loadout Manager opens from **Open Loadout Manager System** on
Arsenal boxes. Its interface is registered as a modal widget with initial focus
on the Personal tab. While the interface is active, native menu input contexts
are refreshed every frame to capture UI input instead of character movement,
look, or weapon input. The text-edit context is active while naming a kit.
Closing it or leaving the box removes the modal and stops the input-context
refresh so gameplay controls resume.
Arsenal interaction prompts are hidden while the manager is open and return
using normal HUD visibility rules after closing. The loadout-name field has a
contrasting background, border, and placeholder.
Save a personal kit first, or switch to Shared to select a server kit; Load is
enabled only when a saved kit is selected.
Personal also discovers GRS named kits in the local `.save` library and legacy
`GRS_Locker/saves` directory, plus `GRS_LockerCrossServer/active.json`. These
entries are prefixed `GRS:` and are read-only: load one and save under a new name
to create a manager-owned copy. GRS files are never changed or indexed by the
manager. Named GRS version-2 kits resolve clothing areas and equipment slot
names against the current character rather than assuming fixed slot indices.
Shared also discovers GRS `sharedkits` files in the server profile's
`GRS_Locker/players/__grs_shared__` directory. These remain read-only even for
manager administrators; the GRS shared library is never modified.
GRS is not a required addon; the equipment addons referenced by a kit must still
be available on the server.
Discovery uses the running application's `$profile:`; Workbench and the game
normally have separate profile directories. Named GRS kits validate their root
slot names and prefabs before replacing equipment; an unavailable root slot or
prefab reports an error without stripping the current kit.

Equipment capture includes subclasses of supported inventory storages (such as
modular vest and pouch storages). Loading reconciles obsolete inventory items
before inserting replacements and continues through item/storage failures so
one unavailable item does not prevent the remaining equipment from loading.
Any failed item or storage is logged and the menu reports a partial-load error,
not success. Kits saved before this fix must be saved again if their stored
data already omitted equipment.

## Feedback

Open a request using the issue templates on this project, or reach out through official unit channels.

---

*All mods are built exclusively for the 506th Infantry Regiment Realism Unit.*
