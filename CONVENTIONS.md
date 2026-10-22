# MetroExodusLike — Conventions du projet

Règles à suivre pour chaque nouvelle fonctionnalité. La roadmap est dans `ROADMAP.md`.

---

## 1. La règle d'or : C++ pour la technique, Blueprint pour le reste

| C++ (`Source/`) | Blueprint / WBP (`Content/`) |
|---|---|
| Systèmes, logique, règles du jeu | Contenu : quel mesh, quel son, quelle animation |
| Calculs, états, machines à états | Réglages : vitesses, durées, seuils (via `UPROPERTY`) |
| Interfaces, composants, classes de base | Mise en page des widgets (UMG) |
| | Placement dans les niveaux |

**Un Blueprint ne contient jamais de logique de gameplay** (pas de nodes dans l'Event Graph).
Exceptions imposées par Unreal : le graphe d'animation (AnimBP) et la mise en page UMG — ils ne font que *lire* des valeurs calculées en C++.

## 2. Ajouter une fonctionnalité : le chemin type

1. **Classe C++** : la base technique (`AMEDoor`, `UMEInteractionComponent`…).
   - Les valeurs réglables sont des `UPROPERTY(EditDefaultsOnly)` ou `EditAnywhere`, avec `Category`, `ClampMin`, `Units`.
   - Les références aux assets (mesh, anim, son, widget) sont des `UPROPERTY` vides en C++ : on ne les remplit jamais en dur (`ConstructorHelpers` interdit).
2. **Blueprint enfant** : la donnée (`BP_Door` hérite de `AMEDoor`) : on y assigne mesh, sons, valeurs.
3. **Dans le monde** : on place le **Blueprint**, jamais la classe C++ directement.
4. **Widgets** : classe C++ (logique) + `WBP_` qui en hérite (mise en page). Les éléments utilisés par le code sont liés avec `UPROPERTY(meta = (BindWidget))` et doivent porter exactement le même nom dans le WBP.
5. **Tester** en jeu, valider, **commit**.

## 3. Nommage

**C++** : préfixe projet `ME` après le préfixe Unreal.

| Type | Exemple |
|---|---|
| Actor | `AMEDoor` |
| Component | `UMEInteractionComponent` |
| Interface | `IMEInteractable` / `UMEInteractable` |
| Struct / Enum | `FMEInteractionInfo` / `EMEInteractionGesture` |
| Log | `UE_LOG(LogME, ...)` |

**Assets**

| Préfixe | Type | Exemple |
|---|---|---|
| `BP_` | Blueprint (enfant d'une classe C++) | `BP_Door`, `BP_MECharacter` |
| `WBP_` | Widget Blueprint | `WBP_InteractionPrompt` |
| `ABP_` | Animation Blueprint | `ABP_MECharacter` |
| `BS_` | Blend Space | `BS_Stand` |
| `A_` | Animation | `A_Walk_Loop_F` |
| `DA_` | Data Asset | `DA_InputConfig` |
| `IA_` / `IMC_` | Input Action / Mapping Context | `IA_Interact`, `IMC_Default` |
| `IK_` / `RTG_` | IK Rig / Retargeter | `RTG_UEFN_to_Quantum` |
| `L_` | Niveau | `L_Test` |
| `SM_` / `SKM_` / `M_` / `MI_` / `T_` | Static mesh / Skeletal mesh / Material / Instance / Texture | |

## 4. Où ranger

| Domaine | C++ (`Source/MetroExodusLike/`) | Content (`/Game/`) |
|---|---|---|
| Règles globales | `Core/` | `Core/` |
| Joueur, caméra, animation | `Player/` | `Player/`, `Player/Animations/` |
| Input | `Input/` | `Input/` |
| Interaction | `Interaction/` | `Interaction/` |
| Interface | `UI/` | `UI/` |
| Armes, IA, survie, inventaire… | un dossier par domaine, créé quand on en a besoin | idem |
| Niveaux | | `Maps/` |

Packs achetés ou importés : dans leur propre dossier (`QuantumCharacter/`, `Characters/UEFN_Mannequin/` = sources GASP). On ne mélange pas nos assets avec ceux des packs.

`Source/MetroExodusLikeEditor/` : outils **éditeur uniquement** (jamais dans le jeu livré), par exemple la construction de WBP par script.

## 5. Style de code

- Commentaires en anglais, courts, qui expliquent le **pourquoi** (pas le quoi).
- Textes affichés au joueur : `LOCTEXT` / `NSLOCTEXT` en anglais, traduits plus tard par la localisation Unreal.
- Pas de `Tick` si ce n'est pas nécessaire ; s'il l'est, on le coupe quand il ne sert plus.
- Délégués C++ (`DECLARE_MULTICAST_DELEGATE`) pour communiquer entre systèmes (ex. interaction → UI → animation).

## 6. Règles de design Metro (décidées)

- **Hybride avec jambes (décidé en Phase 3)** : corps visible = jambes + torse en modules (sans bras ni tête, haut des bras replié), bras FPS + arme FPS sur la caméra, corps du monde complet + arme du monde invisibles (ombre, IA). Actions spéciales (escalade, takedown…) : « moment full body », on affiche le corps du monde.
- **Near clip plane à 2 cm** (`DefaultEngine.ini`) : l'arrière de l'arme est à quelques cm de l'œil.
- **Toujours armé** : pas de touche pour ranger l'arme, sauf zones sûres scriptées.
- **Main gauche** pour toutes les interactions, l'arme reste en main droite.
- **Équipement lourd** : les réceptions de grande hauteur arrêtent net le personnage.
- **HUD minimal** : infos discrètes, le plus possible dans le monde (montre, visière, gestes).

## 7. Workflow

- Une étape de la roadmap à la fois : **code → compile → test en jeu → validation → commit**.
- Compiler avec l'éditeur **fermé** dès qu'un header change (Live Coding seulement pour de petites modifs dans un `.cpp`).
- Assets à acheter : recherche des meilleures options (prix, licence) **avant** tout achat.
