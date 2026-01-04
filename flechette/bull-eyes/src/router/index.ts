// src/router/index.ts
import { createRouter, createWebHistory } from 'vue-router'
import Game from '@/components/Game.vue'
import Home from '@/components/Home.vue'

const routes = [
  { path: '/', name: 'Home', component: Home },
  { path: '/game', name: 'Game', component: Game },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

export default router
