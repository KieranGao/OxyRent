<template>
  <div class="main-layout" :class="{ 'sidebar-collapsed': appStore.sidebarCollapsed }">
    <!-- Sidebar -->
    <aside class="sidebar">
      <div class="sidebar-logo">
        <div class="logo-icon">
          <svg width="20" height="20" viewBox="0 0 32 32" fill="none">
            <path d="M16 2L2 16L16 30L30 16L16 2Z" stroke="currentColor" stroke-width="2" fill="none"/>
            <path d="M16 8L8 16L16 24L24 16L16 8Z" fill="currentColor" opacity="0.4"/>
          </svg>
        </div>
        <span v-if="!appStore.sidebarCollapsed" class="logo-text">OxyRent</span>
      </div>

      <el-menu
        :default-active="activeMenu"
        :collapse="appStore.sidebarCollapsed"
        :collapse-transition="false"
        router
      >
        <el-menu-item index="/dashboard">
          <el-icon><HomeFilled /></el-icon>
          <span>工作台</span>
        </el-menu-item>
        <el-menu-item index="/vehicles">
          <el-icon><Van /></el-icon>
          <span>车辆管理</span>
        </el-menu-item>
        <el-menu-item index="/rentals">
          <el-icon><Document /></el-icon>
          <span>租赁管理</span>
        </el-menu-item>
        <el-menu-item index="/maintenance" v-if="authStore.isStaff || authStore.isAdmin">
          <el-icon><SetUp /></el-icon>
          <span>维保管理</span>
        </el-menu-item>
        <el-menu-item index="/topup" v-if="authStore.isStaff || authStore.isAdmin">
          <el-icon><Wallet /></el-icon>
          <span>充值管理</span>
        </el-menu-item>

        <template v-if="authStore.isAdmin">
          <el-divider style="margin: 12px 16px; border-color: var(--border)" />
          <div v-if="!appStore.sidebarCollapsed" class="menu-group-title">系统管理</div>
          <el-menu-item index="/payments">
            <el-icon><Wallet /></el-icon>
            <span>收费管理</span>
          </el-menu-item>
          <el-menu-item index="/invoices">
            <el-icon><Tickets /></el-icon>
            <span>账单管理</span>
          </el-menu-item>
          <el-menu-item index="/statistics">
            <el-icon><DataLine /></el-icon>
            <span>统计报表</span>
          </el-menu-item>
          <el-menu-item index="/users">
            <el-icon><User /></el-icon>
            <span>用户管理</span>
          </el-menu-item>
        </template>
      </el-menu>

      <div class="sidebar-footer">
        <div v-if="!appStore.sidebarCollapsed" class="version-text">v1.0</div>
      </div>
    </aside>

    <!-- Main Content Area -->
    <div class="main-area">
      <!-- Header -->
      <header class="header">
        <div class="header-left">
          <el-button
            class="collapse-btn"
            :icon="appStore.sidebarCollapsed ? Expand : Fold"
            text
            @click="appStore.toggleSidebar()"
          />
          <el-breadcrumb separator="/">
            <el-breadcrumb-item :to="{ path: '/dashboard' }">首页</el-breadcrumb-item>
            <el-breadcrumb-item v-if="pageTitle">{{ pageTitle }}</el-breadcrumb-item>
          </el-breadcrumb>
        </div>

        <div class="header-right">
          <el-dropdown trigger="click" @command="handleUserCommand">
            <span class="user-info">
              <div class="user-avatar">{{ (authStore.username || 'G')[0].toUpperCase() }}</div>
              <span class="username">{{ authStore.username || 'Guest' }}</span>
              <el-icon class="arrow"><ArrowDown /></el-icon>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile">
                  <el-icon><User /></el-icon> 个人信息
                </el-dropdown-item>
                <el-dropdown-item command="logout" divided>
                  <el-icon><SwitchButton /></el-icon> 退出登录
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </header>

      <!-- Page Content -->
      <main class="content">
        <router-view v-slot="{ Component, route }">
          <transition name="slide-fade" mode="out-in">
            <component :is="Component" :key="route.path" />
          </transition>
        </router-view>
      </main>
    </div>
  </div>
</template>

<script setup>
import { computed, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { useAppStore } from '@/stores/app'
import {
  HomeFilled, Van, Document, SetUp, Wallet, Tickets, DataLine,
  User, Expand, Fold, ArrowDown, SwitchButton,
} from '@element-plus/icons-vue'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()
const appStore = useAppStore()

const activeMenu = computed(() => {
  const path = route.path
  if (path.startsWith('/vehicles')) return '/vehicles'
  if (path.startsWith('/rentals')) return '/rentals'
  if (path.startsWith('/maintenance')) return '/maintenance'
  if (path.startsWith('/payments')) return '/payments'
  if (path.startsWith('/invoices')) return '/invoices'
  if (path.startsWith('/statistics')) return '/statistics'
  if (path.startsWith('/users')) return '/users'
  return path
})
const pageTitle = computed(() => route.meta?.title || '')

watch(pageTitle, (title) => {
  appStore.setPageTitle(title)
}, { immediate: true })

function handleUserCommand(cmd) {
  if (cmd === 'profile') {
    router.push('/profile')
  } else if (cmd === 'logout') {
    authStore.logout()
  }
}
</script>

<style scoped>
.main-layout {
  display: flex;
  height: 100vh;
  overflow: hidden;
  background: var(--bg-primary);
}

/* ===== Sidebar ===== */
.sidebar {
  width: var(--sidebar-width);
  min-width: var(--sidebar-width);
  background: var(--bg-sidebar);
  display: flex;
  flex-direction: column;
  transition: width 0.3s var(--ease), min-width 0.3s var(--ease);
  overflow: hidden;
  position: relative;
  border-right: 1px solid var(--border);
}

.sidebar-collapsed .sidebar {
  width: var(--sidebar-collapsed);
  min-width: var(--sidebar-collapsed);
}

.sidebar-logo {
  height: var(--header-height);
  display: flex;
  align-items: center;
  padding: 0 20px;
  gap: 12px;
  border-bottom: 1px solid var(--border);
}

.sidebar-logo .logo-icon {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--accent);
  flex-shrink: 0;
}

.sidebar-logo .logo-text {
  font-family: var(--font-display);
  font-size: 20px;
  font-weight: 500;
  color: var(--text-primary);
  letter-spacing: 1.5px;
  white-space: nowrap;
}

.menu-group-title {
  padding: 20px 20px 8px;
  font-size: 10px;
  color: var(--text-tertiary);
  text-transform: uppercase;
  letter-spacing: 2px;
  font-weight: 600;
}

.sidebar :deep(.el-menu) {
  border-right: none;
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  padding: 8px 0;
}

.sidebar :deep(.el-menu-item) {
  height: 40px;
  line-height: 40px;
  margin: 2px 10px;
  border-radius: var(--radius-sm);
  font-size: 13px;
  font-weight: 400;
  color: var(--text-secondary);
  transition: all 0.2s var(--ease);
}

.sidebar :deep(.el-menu-item:hover) {
  background: rgba(255, 255, 255, 0.04) !important;
  color: var(--text-primary);
}

.sidebar :deep(.el-menu-item.is-active) {
  background: var(--accent-muted) !important;
  color: var(--accent) !important;
  font-weight: 500;
}

.sidebar :deep(.el-menu-item.is-active::before) {
  content: '';
  position: absolute;
  left: 0;
  top: 50%;
  transform: translateY(-50%);
  width: 2px;
  height: 16px;
  background: var(--accent);
  border-radius: 0 2px 2px 0;
}

.sidebar :deep(.el-divider--horizontal) {
  margin: 12px 16px;
  border-color: var(--border);
}

.sidebar-footer {
  padding: 16px 20px;
  border-top: 1px solid var(--border);
}

.version-text {
  font-size: 10px;
  color: var(--text-tertiary);
  letter-spacing: 1px;
  text-align: center;
}

/* ===== Main Area ===== */
.main-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}

/* ===== Header ===== */
.header {
  height: var(--header-height);
  min-height: var(--header-height);
  background: rgba(5, 5, 5, 0.8);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border-bottom: 1px solid var(--border);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 28px;
  position: sticky;
  top: 0;
  z-index: 10;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 14px;
}

.collapse-btn {
  color: var(--text-secondary);
  font-size: 18px;
}
.collapse-btn:hover {
  color: var(--text-primary);
}

.header-right {
  display: flex;
  align-items: center;
  gap: 16px;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 10px;
  cursor: pointer;
  padding: 6px 12px;
  border-radius: var(--radius-md);
  transition: background 0.2s var(--ease);
}

.user-info:hover {
  background: rgba(255, 255, 255, 0.04);
}

.user-avatar {
  width: 30px;
  height: 30px;
  border-radius: 50%;
  background: var(--accent-muted);
  color: var(--accent);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  font-weight: 600;
  font-family: var(--font-body);
}

.username {
  font-size: 13px;
  color: var(--text-primary);
  font-weight: 500;
}

.arrow {
  font-size: 12px;
  color: var(--text-tertiary);
  transition: transform 0.2s;
}

/* ===== Content ===== */
.content {
  flex: 1;
  overflow-y: auto;
  background: var(--bg-primary);
}

/* ===== Responsive ===== */
.sidebar-collapsed .sidebar-logo {
  justify-content: center;
  padding: 0;
}
</style>