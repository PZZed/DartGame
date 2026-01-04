<!-- src/views/Game.vue -->
<template>
  <v-container class="pa-4" fluid>
    <!-- Nom du jeu en haut -->
    <v-row class="mb-6" justify="center">
      <v-col class="text-center" cols="12">
        <h1 class="game-title">{{ store.game.gameName() }}</h1>
      </v-col>
    </v-row>

    <!-- Blocs des joueurs -->
    <v-row dense justify="center">
      <v-col
        v-for="(player, index) in store.players"
        :key="player.name"
        cols="12"
        lg="3"
        md="4"
        sm="6"
      >
        <v-card
          class="pa-6 text-center player-card"
          :class="{ 'active-player': currentPlayerIndex === index }"
          elevation="6"
        >
          <div class="player-name">{{ player.name }}</div>
          <div class="player-score">{{ player.score }}</div>

          <div>{{ player.darts }}</div>
          <!-- Fléchettes du joueur actif -->
          <div v-if="currentPlayerIndex === index" class="darts-row mt-4">
            <div
              v-for="i in store.thrownDart"
              :key="i"
            >
              🎯
            </div>
          </div>
        </v-card>
      </v-col>
    </v-row>

    <!-- Boutons en bas -->
    <v-row class="mt-8" justify="center">
      <v-col class="d-flex justify-center" cols="12" md="6" sm="8">
        <v-btn
          class="ma-4 huge-btn"
          color="red darken-1"
          @click="throwMiss"
        >
          MISS
        </v-btn>
        <v-btn
          class="ma-4 huge-btn"
          color="green darken-1"
          @click="nextPlayer"
        >
          Joueur suivant
        </v-btn>
        <v-btn
          class="ma-4 huge-btn"
          color="green darken-1"
          @click="menuPrincipal"
        >
          Menu Principal
        </v-btn>
      </v-col>
    </v-row>
  </v-container>
</template>

<script lang="ts" setup>
  import { ref } from 'vue'
  import { useRouter } from 'vue-router'
  import { useGameStore } from '@/stores/game'
  const router = useRouter()
  const store = useGameStore()
  const ws = new WebSocket('ws://192.168.1.20:81')

  ws.addEventListener('message', event => {
    const json = JSON.parse(event.data)
    store.throwDart(json.score)
  })
  ws.addEventListener('error', event => {
    alert('Impossible de se connecter à la websocket')
  })

  store.startGame()

  // Joueur actuel
  const currentPlayerIndex = ref(0)
  // Nombre de fléchettes déjà lancées dans ce tour (0-3)
  // MISS = on lance une fléchette mais score 0
  function throwMiss () {
    // tu peux ici ajouter un score de 0 au joueur actif si besoin
    store.throwDart('0')
  }

  function nextPlayer () {
    currentPlayerIndex.value = store.nextPlayer()
  }
  function menuPrincipal () {
    router.push('/')
  }

</script>

<style scoped>
.game-title {
  font-size: 3rem;
  font-weight: bold;
  color: #1976d2;
}

.player-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 220px;
  border-radius: 16px;
  transition: transform 0.2s, box-shadow 0.2s;
}

.active-player {
  border: 4px solid #ffeb3b;
  transform: scale(1.05);
}

.player-name {
  font-size: 1.5rem;
  font-weight: bold;
  margin-bottom: 1rem;
}

.player-score {
  font-size: 4rem;
  font-weight: bold;
  color: #e53935;
}

.darts-row {
  display: flex;
  justify-content: center;
  gap: 12px;
}

.v-icon {
  font-size: 2.5rem;
}

</style>
