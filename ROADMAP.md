# MetroExodusLike — Roadmap Vertical Slice

**Objectif :** une vertical slice solo, jouable du début à la fin, d'environ 15-20 min, qui donne « l'effet Metro ».
Un tunnel + une station, des humains et quelques mutants, la survie avec masque à gaz, l'infiltration à la lumière, le crafting de base.

**Règles du projet**
- Toute la logique en C++. Les Blueprints servent **uniquement** de fichiers de données (mesh, sons, valeurs).
- Une étape = on code → on compile → **on teste en jeu** → on valide le critère « Fini quand » → étape suivante.
- On ne commence jamais une étape tant que la précédente n'est pas validée.
- Greybox (cubes) d'abord, assets achetés seulement quand une mécanique fonctionne.
- Solo uniquement, pas de réseau.

---

## Phase 0 — Fondations
- [x] 0.1 Git + `.gitignore` Unreal (+ LFS pour `Content/`)
- [x] 0.2 Arborescence `Source/` par domaine (Core, Player créés ; Weapons, AI, Survival, Interaction, Inventory créés quand on en a besoin)
- [x] 0.3 Build.cs : modules nécessaires (EnhancedInput, AIModule, GameplayTasks, NavigationSystem, UMG, PhysicsCore) + log `LogME`
- [x] 0.4a `AMEGameMode`, `AMEPlayerController`, `AMECharacter` (caméra FP) + GameMode par défaut dans la config
- [x] 0.4b Map de test greybox `Content/Maps/L_Test` (tunnel, escalier, rampe, caisses de saut, passage bas)

**Fini quand :** le projet compile, PIE démarre sur la map de test avec notre GameMode.

## Phase 1 — Le corps du joueur (true FPS)
Choix : **true FPS** — un seul corps complet (`SKM_QuantumCharacter_NoHead`), caméra dans la tête, on voit ses jambes.
Réévaluation en Phase 3 : rester en true FPS pur ou passer en hybride (bras FPS `SKM_Arms` pour les armes).

- [x] 1.1 Enhanced Input : Input Actions + Mapping Context (assets) référencés par un DataAsset, bindés en C++
- [x] 1.2 Marcher / courir / sauter / regarder (testable avec la capsule seule)
- [x] 1.3 Corps true FPS : mesh NoHead, caméra sur le bone de la tête, `UMEAnimInstance` en C++ + AnimBP (graphe uniquement)
- [x] 1.4 Stabilisation de la caméra (filtrer le mouvement de tête de l'anim)
- [x] 1.5 Locomotion complète via Game Animation Sample (gratuit, retargeté) : marche arrière, strafe, rotation sur place
- [x] 1.6 S'accroupir (transition de caméra fluide)
- [ ] 1.7 Poids Metro : inertie, inclinaison légère de caméra
- [ ] 1.8 Bruits de pas selon la surface (Physical Materials + anim notifies)

**Fini quand :** se déplacer est agréable et « lourd », et on voit son corps de façon crédible en regardant en bas.

## Phase 2 — Interaction
- [ ] 2.1 Interface `IMEInteractable` + traces depuis la caméra
- [ ] 2.2 Prompt minimal à l'écran (petit texte contextuel)
- [ ] 2.3 Premiers interactables : porte, objet ramassable, interrupteur

**Fini quand :** on peut ouvrir une porte et ramasser un objet.

## Phase 3 — Première arme
- [ ] 3.1 `AMEWeaponBase` + `UMEWeaponData` (DataAsset : dégâts, cadence, chargeur…)
- [ ] 3.2 Bras first-person + arme en main (placeholder)
- [ ] 3.3 Tir hitscan, munitions, rechargement
- [ ] 3.4 Recul, dispersion, visée (ADS)
- [ ] 3.5 Cibles avec santé (`UMEHealthComponent`, réutilisable partout)
- [ ] 3.6 Impacts : decals, particules, sons

**Fini quand :** tirer sur des cibles est satisfaisant.
💰 **Achat d'assets :** pack bras FPS + animations d'armes (l'achat le plus important du projet).

## Phase 4 — Survie
- [ ] 4.1 Santé joueur, dégâts, mort / respawn
- [ ] 4.2 Medkit (raccourci, animation)
- [ ] 4.3 Zones toxiques (volumes)
- [ ] 4.4 Masque à gaz : mettre / retirer, filtres avec chrono, essoufflement
- [ ] 4.5 Visière : buée / fissures (post-process), essuyer la visière
- [ ] 4.6 Montre au poignet : temps de filtre restant + indicateur de visibilité (branché en Phase 6)

**Fini quand :** traverser une zone toxique est stressant et lisible sans HUD.

## Phase 5 — IA humaine
- [ ] 5.1 `AMEAICharacter` + `AMEAIController` (Behavior Tree ou StateTree, à décider à ce moment)
- [ ] 5.2 Perception : vue + ouïe (tirs, pas)
- [ ] 5.3 États : patrouille → suspicion → alerte → combat → recherche
- [ ] 5.4 Combat : tir, mise à couvert simple, déplacement
- [ ] 5.5 Mort, ragdoll, loot sur le corps

**Fini quand :** un groupe de 3 ennemis offre un combat crédible.

## Phase 6 — Lumière et infiltration
- [ ] 6.1 Calcul de visibilité du joueur (lumières proches + ombre)
- [ ] 6.2 Indicateur sur la montre (LED)
- [ ] 6.3 Lampe torche (batterie + recharge manuelle) et briquet
- [ ] 6.4 Lumières destructibles / éteignables
- [ ] 6.5 La perception IA utilise la visibilité du joueur
- [ ] 6.6 Takedown silencieux (assommer / tuer) par derrière

**Fini quand :** on peut nettoyer une zone sans être repéré, en jouant avec l'ombre.

## Phase 7 — Inventaire et crafting
- [ ] 7.1 Ressources (communes / techniques) + munitions + consommables
- [ ] 7.2 Loot dans le monde et sur les corps
- [ ] 7.3 Menu sac à dos (crafting filtres, medkits, munitions de base)
- [ ] 7.4 Nettoyage d'arme / encrassement (optionnel slice)

**Fini quand :** la boucle explorer → looter → crafter → survivre tourne.

## Phase 8 — Mutants
- [ ] 8.1 Mutant de mêlée (bondit, attaque, fuit la lumière ?)
- [ ] 8.2 Comportement de meute simple

**Fini quand :** une rencontre mutants dans le tunnel fait peur.

## Phase 9 — Construire la slice
- [ ] 9.1 Level design final : tunnel → zone toxique → station occupée → sortie
- [ ] 9.2 Sauvegarde / checkpoints
- [ ] 9.3 Menu principal, pause, options (sensibilité, graphismes, volume)
- [ ] 9.4 Audio d'ambiance, musique, éclairage final
- [ ] 9.5 Optimisation (profiling Unreal Insights, LODs, culling)
- [ ] 9.6 Playtests + corrections

**Fini quand :** quelqu'un qui n'a jamais vu le projet le termine et en redemande.

---

## Hors slice (plus tard, si financement)
Customisation d'armes sur établi, cycle jour/nuit, niveaux ouverts, véhicule, dialogues / narration, PNJ alliés, plus d'armes et de mutants.
