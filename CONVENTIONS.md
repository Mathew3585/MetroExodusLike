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

- **Hybride (décidé en Phase 3)**, personnage **MetaHuman** (`Content/Characters/Player/`), monté comme la démo du pack AR-15 :
  - **Vue FPS** : `FirstPersonPivot` (hauteur des yeux, tourné avec la vue + head bob/respiration) → **bras FPS** du pack (squelette Mannequin) → **caméra sur l'os `head` des bras**. Les animations FPS du pack sont jouées **telles quelles** (jamais retargetées) : le cadrage et les mouvements de caméra sont ceux de l'animateur. L'arme FPS est sur `ik_hand_gun`.
  - **Bras FPS = bras MetaHuman sur le squelette du pack** (`SKM_Player_FPArms_Body/Outfit`) : export FBX des bras MH découpés + os `ik_*`/`interaction`/`center_of_mass` du pack, import sur `SK_Mannequin` (les os MH en plus y sont fusionnés, **ne jamais les supprimer** : leurs poids de skin seraient perdus). Translation retargeting de `SK_Mannequin` en **Animation** partout : les bras prennent les longueurs d'os du pack, mains et arme tombent comme l'animateur l'a prévu. À refaire si le pack est réimporté.
  - **Corps visible** = buste + jambes (`SKM_Player_Legs_*`, sans bras ni tête), sans ombre. **Corps du monde** = MetaHuman complet invisible (ombre, IA) avec l'arme du monde. Actions spéciales (escalade, takedown…) : « moment full body », on affiche le corps du monde.
  - Regard vers le bas limité (`MinViewPitch`), caméra avancée (`EyeOffset`) et corps qui suit la vue quand on regarde en bas : le buste reste sous les bras.
- **Near clip plane à 1 cm** (`DefaultEngine.ini`) + **`FirstPersonScale` à 0.25** (caméra FPS) : bras et arme écrasés vers l'œil (même image à l'écran, seule la profondeur change), ils ne rentrent plus dans les murs. Tout ce qui part de l'arme vers le monde (douilles…) part de la position visible (`GetFirstPersonVisualLocation`). Gestes main gauche sur un objet du monde : remonter le scale vers 1 pendant le geste.
- **Animations du corps MetaHuman** (corps visible et du monde) : toujours **retargetées** sur `metahuman_base_skel` (jamais « compatible skeletons » : orientations d'os différentes). Le squelette MetaHuman est partagé, les proportions viennent du mesh : mesurer les poses avec `optional_skeletal_mesh`. Pas d'os `ik_*` : l'arme est tenue par `hand_r` + `GripOffset` (DataAsset de l'arme).
- **Toujours armé** : pas de touche pour ranger l'arme, sauf zones sûres scriptées.
- **Main gauche** pour toutes les interactions, l'arme reste en main droite.
- **Équipement lourd** : les réceptions de grande hauteur arrêtent net le personnage.
- **HUD minimal** : infos discrètes, le plus possible dans le monde (montre, visière, gestes).

## 7. Workflow

- Une étape de la roadmap à la fois : **code → compile → test en jeu → validation → commit**.
- Compiler avec l'éditeur **fermé** dès qu'un header change (Live Coding seulement pour de petites modifs dans un `.cpp`).
- Assets à acheter : recherche des meilleures options (prix, licence) **avant** tout achat.
