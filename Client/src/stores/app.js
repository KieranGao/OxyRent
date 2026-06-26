import { defineStore } from 'pinia'
import { ref, watch } from 'vue'

export const useAppStore = defineStore('app', () => {
  const savedTheme = localStorage.getItem('theme')
  const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches
  const theme = ref(savedTheme || (prefersDark ? 'dark' : 'light'))
  const sidebarCollapsed = ref(false)
  const pageTitle = ref('')

  function toggleTheme() {
    theme.value = theme.value === 'light' ? 'dark' : 'light'
  }

  function toggleSidebar() {
    sidebarCollapsed.value = !sidebarCollapsed.value
  }

  function setPageTitle(title) {
    pageTitle.value = title
    document.title = title ? `${title} - OxyRent` : 'OxyRent'
  }

  function applyTheme() {
    document.documentElement.classList.toggle('dark', theme.value === 'dark')
  }

  watch(theme, (val) => {
    localStorage.setItem('theme', val)
    applyTheme()
  })

  applyTheme()

  return {
    theme, sidebarCollapsed, pageTitle,
    toggleTheme, toggleSidebar, setPageTitle, applyTheme,
  }
})
