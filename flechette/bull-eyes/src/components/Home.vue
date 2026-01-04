<template>
  <v-container class="pa-4">
    <v-card class="pa-4" max-width="500">
      <v-card-title>Nouvelle Partie</v-card-title>
      <v-card-text>
        <v-select v-model="gameType" :items="gameTypes" label="Type de jeu" required />
        <v-select v-model="numPlayers" :items="playerNumbers" label="Nombre de joueurs" required />

        <div v-for="i in numPlayers" :key="i" class="mt-2">
          <v-text-field v-model="playerNames[i - 1]" :label="`Nom du joueur ${i}`" required />
        </div>
      </v-card-text>
      <v-card-actions>
        <v-btn color="primary" :disabled="!canStart" @click="startGame">Valider</v-btn>
      </v-card-actions>
    </v-card>
  </v-container>
</template>

<script lang="ts" setup>
  import { computed, ref } from 'vue'
  import { useRouter } from 'vue-router'
  import { useGameStore } from '@/stores/game'

  const router = useRouter()
  const store = useGameStore()

  const gameTypes = ['301', 'Golf']
  const playerNumbers = Array.from({ length: 8 }, (_, i) => i + 1)

  const gameType = ref('')
  const numPlayers = ref(2)
  const playerNames = ref<string[]>(Array.from({ length: 8 }).fill(''))

  const canStart = computed(() =>
    gameType.value
    && playerNames.value.slice(0, numPlayers.value).every(name => name.trim() !== ''),
  )

  function startGame () {
    store.setGame(gameType.value, playerNames.value.slice(0, numPlayers.value))
    router.push('/game')
  }
</script>
